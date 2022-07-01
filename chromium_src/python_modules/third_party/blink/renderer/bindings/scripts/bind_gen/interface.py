# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import re

import lib.override_utils as override_utils

_IS_OBSERVABLE_ARRAY_SETTER = "pg_is_observable_array_setter"

_PAGE_GRAPH_TRACKED_ITEMS = {
    "OffscreenCanvasRenderingContext2D": {
        "measureText",
    },
    "CanvasRenderingContext2D": {
        "measureText",
    },
    "Document": {
        "cookie",
        "referrer",
    },
    "HTMLCanvasElement": {
        "getContext",
        "toBlob",
        "toDataURL",
    },
    "Navigator": {"*"},
    "Performance": {"*"},
    "PerformanceObserver": {"*"},
    "PerformanceNavigation": {"*"},
    "PerformanceTiming": {"*"},
    "Screen": {"*"},
    "Storage": {"*"},
    "Window": {
        "localStorage",
        "performance",
        "sessionStorage",
        "fetch",
    },
    "WorkerGlobalScope": {
        "performance",
        "fetch",
    },
    "WebGLRenderingContext": {
        "getExtension",
        "getParameter",
        "getShaderPrecisionFormat",
    },
    "WebGL2RenderingContext": {
        "getExtension",
        "getParameter",
    },
    "XMLHttpRequest": {"*"},
    "XMLHttpRequestEventTarget": {"*"},
    "XMLHttpRequestUpload": {"*"},
}

### Helpers begin.


def _should_track_in_page_graph(cg_context):
    if not cg_context.property_:
        return False

    return True

    tracked_class_items = _PAGE_GRAPH_TRACKED_ITEMS.get(
        cg_context.class_like.identifier, None)
    if tracked_class_items is None or len(tracked_class_items) == 0:
        return False

    if "*" in tracked_class_items:
        return True

    return cg_context.property_.identifier in tracked_class_items


def _create_single_node(*args):
    nodes = [node for node in args if node]
    if not nodes:
        return None
    if len(nodes) == 1:
        return nodes[0]
    return SequenceNode(nodes)


def _to_page_graph_blink_receiver_data(arg):
    return "ToPageGraphBlinkReceiverData({})".format(arg)


def _to_page_graph_blink_arg(arg):
    return "ToPageGraphBlinkArg({})".format(arg)


### Helpers end.


def _make_report_page_graph_binding_event(cg_context):
    assert isinstance(cg_context, CodeGenContext)
    if not _should_track_in_page_graph(cg_context):
        return None

    if cg_context.attribute_get or cg_context.exposed_construct:
        binding_type = "Attribute"
        binding_event = "AttributeGet"
    elif cg_context.attribute_set:
        binding_type = "Attribute"
        binding_event = "AttributeSet"
    elif cg_context.constant:
        binding_type = "Constant"
        binding_event = "ConstantGet"
    elif cg_context.constructor_group:
        binding_type = "Constructor"
        binding_event = "ConstructorCall"
    elif cg_context.operation_group or cg_context.stringifier:
        binding_type = "Method"
        binding_event = "MethodCall"
    else:
        assert False, "PageGraph unsupported binding type for: {}.{}".format(
            cg_context.class_like.identifier, cg_context.property_.identifier)

    pattern = ("if (UNLIKELY(${page_graph_enabled})) {{\n"
               "  probe::RegisterPageGraphBindingEvent("
               "${execution_context}, ${page_graph_binding_name}, "
               "::brave_page_graph::kBindingType{_1}, "
               "::brave_page_graph::kBindingEvent{_2});\n"
               "}}")
    _1 = binding_type
    _2 = binding_event
    return TextNode(_format(pattern, _1=_1, _2=_2))


def _bind_page_graph_local_vars(code_node, cg_context):
    assert isinstance(cg_context, CodeGenContext)
    if not _should_track_in_page_graph(cg_context):
        return

    page_graph_enabled_node = SymbolNode(
        "page_graph_enabled",
        ("const bool ${page_graph_enabled} = "
         "CoreProbeSink::HasAgentsGlobal(CoreProbeSink::kPageGraphAgent);"))
    page_graph_enabled_node.accumulate(
        CodeGenAccumulator.require_include_headers([
            "brave/third_party/blink/brave_page_graph/blink_converters.h",
            "brave/third_party/blink/brave_page_graph/types.h",
            "third_party/blink/renderer/core/probe/core_probes.h",
        ]))
    code_node.register_code_symbol(page_graph_enabled_node)

    # Event name format is used from make_bindings_trace_event to not duplicate
    # strings in the final binary.
    event_name = "{}.{}".format(cg_context.class_like.identifier,
                                cg_context.property_.identifier)
    if cg_context.attribute_get:
        event_name = "{}.{}".format(event_name, "get")
    elif cg_context.attribute_set:
        event_name = "{}.{}".format(event_name, "set")
    elif cg_context.constructor_group and not cg_context.is_named_constructor:
        event_name = "{}.{}".format(cg_context.class_like.identifier,
                                    "constructor")

    page_graph_binding_name_node = SymbolNode(
        "page_graph_binding_name",
        _format("constexpr char ${page_graph_binding_name}[] = \"{_1}\";",
                _1=event_name))

    code_node.register_code_symbol(page_graph_binding_name_node)


@override_utils.override_function(globals())
def bind_callback_local_vars(original_function, code_node, cg_context):
    original_function(code_node, cg_context)
    _bind_page_graph_local_vars(code_node, cg_context)


@override_utils.override_function(globals())
def _make_empty_callback_def(original_function, cg_context, function_name):
    func_def = original_function(cg_context, function_name)
    body = func_def.body
    body.extend([
        _make_report_page_graph_binding_event(cg_context),
    ])
    return func_def


def _append_report_page_graph_api_call_event(cg_context, expr):
    assert isinstance(cg_context, CodeGenContext)
    if not _should_track_in_page_graph(cg_context):
        return expr

    if cg_context.no_alloc_direct_call_for_testing:
        return expr
    assert "\n" not in expr

    # Extract blink_receiver and args from a string like:
    # ${blink_receiver}->getExtension(${script_state}, ${arg1_name})
    if expr.startswith("${blink_receiver}"):
        receiver_data = _to_page_graph_blink_receiver_data("${blink_receiver}")
    else:
        receiver_data = "{}"

    args = re.findall((r"(\${(?:"
                       r"arg\d+\w+|blink_property_name|blink_property_value"
                       r")})"), expr)
    args = ", ".join(map(_to_page_graph_blink_arg, args))

    if cg_context.may_throw_exception:
        exception_state = "&${exception_state}"
    else:
        exception_state = "nullptr"

    is_return_type_void = ((not cg_context.return_type
                            or cg_context.return_type.unwrap().is_void)
                           and not cg_context.does_override_idl_return_type)
    if is_return_type_void or hasattr(cg_context, _IS_OBSERVABLE_ARRAY_SETTER):
        return_value = "absl::nullopt"
    else:
        return_value = _to_page_graph_blink_arg("return_value")

    pattern = (
        ";\n"
        "if (UNLIKELY(${page_graph_enabled})) {{\n"
        "  probe::RegisterPageGraphWebAPICallWithResult(${execution_context}, "
        "${page_graph_binding_name}, {_1}, {{{_2}}}, {_3}, {_4});\n"
        "}}")
    _1 = receiver_data
    _2 = args
    _3 = exception_state
    _4 = return_value
    expr += _format(pattern, _1=_1, _2=_2, _3=_3, _4=_4)
    return expr


@override_utils.override_function(globals())
def _make_blink_api_call(original_function,
                         code_node,
                         cg_context,
                         num_of_args=None,
                         overriding_args=None):
    expr = original_function(code_node, cg_context, num_of_args,
                             overriding_args)
    return _append_report_page_graph_api_call_event(cg_context, expr)


@override_utils.override_function(globals())
def make_attribute_set_callback_def(original_function, cg_context,
                                    function_name):
    is_observable_array = cg_context.attribute.idl_type.unwrap(
        typedef=True).is_observable_array
    if is_observable_array:
        setattr(cg_context, _IS_OBSERVABLE_ARRAY_SETTER, True)

    result = original_function(cg_context, function_name)

    if is_observable_array:
        delattr(cg_context, _IS_OBSERVABLE_ARRAY_SETTER)
    return result
