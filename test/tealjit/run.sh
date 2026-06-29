#!/bin/sh
set -eu

LUAJIT=${LUAJIT:-../../src/luajit}

"$LUAJIT" -e 'local m = dofile("export_record.tl"); assert(m.add(20, 22) == 42)'
"$LUAJIT" -e 'local m = dofile("export_record.tl"); m.add = m.add; local ok, err = pcall(function() m.extra = 1 end); assert(not ok and tostring(err):match("frozen record"))'
"$LUAJIT" -t strict=off -e 'local m = dofile("export_record.tl"); m.extra = 1; assert(m.extra == 1)'

if "$LUAJIT" strict_nil_bad.tl >/tmp/tealjit-strict-nil.out 2>&1; then
  echo "strict_nil_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "strict nil safety" /tmp/tealjit-strict-nil.out

"$LUAJIT" strict_off_nil.tl
"$LUAJIT" -t strict=off strict_cli_off.tl
"$LUAJIT" is_primitive.tl
"$LUAJIT" record_fields.tl
"$LUAJIT" function_calls.tl
"$LUAJIT" function_type_annotations.tl
"$LUAJIT" record_method_colon_call.tl
"$LUAJIT" type_aliases.tl
"$LUAJIT" global_declarations.tl
"$LUAJIT" table_types.tl
"$LUAJIT" for_iterators.tl

if "$LUAJIT" as_bad.tl >/tmp/tealjit-as-bad.out 2>&1; then
  echo "as_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "invalid strict cast" /tmp/tealjit-as-bad.out

if "$LUAJIT" is_name_only_bad.tl >/tmp/tealjit-is-name-only.out 2>&1; then
  echo "is_name_only_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "can only use 'is' on variables" /tmp/tealjit-is-name-only.out

if "$LUAJIT" record_field_unknown_read_bad.tl >/tmp/tealjit-record-read.out 2>&1; then
  echo "record_field_unknown_read_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "unknown record field" /tmp/tealjit-record-read.out

if "$LUAJIT" record_field_unknown_write_bad.tl >/tmp/tealjit-record-write.out 2>&1; then
  echo "record_field_unknown_write_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "unknown record field" /tmp/tealjit-record-write.out

if "$LUAJIT" record_field_type_bad.tl >/tmp/tealjit-record-type.out 2>&1; then
  echo "record_field_type_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "record field type mismatch" /tmp/tealjit-record-type.out

if "$LUAJIT" function_call_arity_bad.tl >/tmp/tealjit-call-arity.out 2>&1; then
  echo "function_call_arity_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "function arity mismatch" /tmp/tealjit-call-arity.out

if "$LUAJIT" function_call_type_bad.tl >/tmp/tealjit-call-type.out 2>&1; then
  echo "function_call_type_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "function argument type mismatch" /tmp/tealjit-call-type.out

if "$LUAJIT" function_call_upvalue_type_bad.tl >/tmp/tealjit-call-upvalue-type.out 2>&1; then
  echo "function_call_upvalue_type_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "function argument type mismatch" /tmp/tealjit-call-upvalue-type.out

if "$LUAJIT" function_call_return_bad.tl >/tmp/tealjit-call-return.out 2>&1; then
  echo "function_call_return_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "type mismatch" /tmp/tealjit-call-return.out

if "$LUAJIT" record_function_call_type_bad.tl >/tmp/tealjit-record-call-type.out 2>&1; then
  echo "record_function_call_type_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "function argument type mismatch" /tmp/tealjit-record-call-type.out

if "$LUAJIT" function_type_annotation_arg_bad.tl >/tmp/tealjit-ftype-arg.out 2>&1; then
  echo "function_type_annotation_arg_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "function argument type mismatch" /tmp/tealjit-ftype-arg.out

if "$LUAJIT" function_type_annotation_arity_bad.tl >/tmp/tealjit-ftype-arity.out 2>&1; then
  echo "function_type_annotation_arity_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "function arity mismatch" /tmp/tealjit-ftype-arity.out

if "$LUAJIT" function_type_annotation_assign_bad.tl >/tmp/tealjit-ftype-assign.out 2>&1; then
  echo "function_type_annotation_assign_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "type mismatch" /tmp/tealjit-ftype-assign.out

if "$LUAJIT" record_function_type_assignment_bad.tl >/tmp/tealjit-record-ftype-assign.out 2>&1; then
  echo "record_function_type_assignment_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "record field type mismatch" /tmp/tealjit-record-ftype-assign.out

if "$LUAJIT" type_alias_primitive_bad.tl >/tmp/tealjit-alias-primitive.out 2>&1; then
  echo "type_alias_primitive_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "type mismatch" /tmp/tealjit-alias-primitive.out

if "$LUAJIT" type_alias_nil_bad.tl >/tmp/tealjit-alias-nil.out 2>&1; then
  echo "type_alias_nil_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "strict nil safety" /tmp/tealjit-alias-nil.out

if "$LUAJIT" type_alias_function_call_bad.tl >/tmp/tealjit-alias-fcall.out 2>&1; then
  echo "type_alias_function_call_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "function argument type mismatch" /tmp/tealjit-alias-fcall.out

if "$LUAJIT" type_alias_param_bad.tl >/tmp/tealjit-alias-param.out 2>&1; then
  echo "type_alias_param_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "function argument type mismatch" /tmp/tealjit-alias-param.out

if "$LUAJIT" type_alias_function_assign_bad.tl >/tmp/tealjit-alias-fassign.out 2>&1; then
  echo "type_alias_function_assign_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "type mismatch" /tmp/tealjit-alias-fassign.out

if "$LUAJIT" type_alias_record_field_bad.tl >/tmp/tealjit-alias-record.out 2>&1; then
  echo "type_alias_record_field_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "record field type mismatch" /tmp/tealjit-alias-record.out

if "$LUAJIT" type_alias_duplicate_bad.tl >/tmp/tealjit-alias-duplicate.out 2>&1; then
  echo "type_alias_duplicate_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "duplicate type alias" /tmp/tealjit-alias-duplicate.out

if "$LUAJIT" type_alias_builtin_bad.tl >/tmp/tealjit-alias-builtin.out 2>&1; then
  echo "type_alias_builtin_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "type alias shadows builtin" /tmp/tealjit-alias-builtin.out

if "$LUAJIT" bare_type_bad.tl >/tmp/tealjit-bare-type.out 2>&1; then
  echo "bare_type_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "types need to be declared" /tmp/tealjit-bare-type.out

if "$LUAJIT" bare_record_bad.tl >/tmp/tealjit-bare-record.out 2>&1; then
  echo "bare_record_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "record needs to be declared" /tmp/tealjit-bare-record.out

if "$LUAJIT" global_decl_type_bad.tl >/tmp/tealjit-global-decl-type.out 2>&1; then
  echo "global_decl_type_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "global type mismatch" /tmp/tealjit-global-decl-type.out

if "$LUAJIT" global_assign_type_bad.tl >/tmp/tealjit-global-assign-type.out 2>&1; then
  echo "global_assign_type_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "global type mismatch" /tmp/tealjit-global-assign-type.out

if "$LUAJIT" global_function_call_bad.tl >/tmp/tealjit-global-call.out 2>&1; then
  echo "global_function_call_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "function argument type mismatch" /tmp/tealjit-global-call.out

if "$LUAJIT" global_function_dotted_bad.tl >/tmp/tealjit-global-func-dotted.out 2>&1; then
  echo "global_function_dotted_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "global function must use a simple name" /tmp/tealjit-global-func-dotted.out

if "$LUAJIT" global_record_field_bad.tl >/tmp/tealjit-global-record-field.out 2>&1; then
  echo "global_record_field_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "record field type mismatch" /tmp/tealjit-global-record-field.out

if "$LUAJIT" global_record_unknown_bad.tl >/tmp/tealjit-global-record-unknown.out 2>&1; then
  echo "global_record_unknown_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "unknown record field" /tmp/tealjit-global-record-unknown.out

if "$LUAJIT" table_array_value_bad.tl >/tmp/tealjit-table-array-value.out 2>&1; then
  echo "table_array_value_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "type mismatch" /tmp/tealjit-table-array-value.out

if "$LUAJIT" table_map_key_bad.tl >/tmp/tealjit-table-map-key.out 2>&1; then
  echo "table_map_key_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "type mismatch" /tmp/tealjit-table-map-key.out

if "$LUAJIT" table_map_value_bad.tl >/tmp/tealjit-table-map-value.out 2>&1; then
  echo "table_map_value_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "type mismatch" /tmp/tealjit-table-map-value.out

if "$LUAJIT" table_index_key_bad.tl >/tmp/tealjit-table-index-key.out 2>&1; then
  echo "table_index_key_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "table key type mismatch" /tmp/tealjit-table-index-key.out

if "$LUAJIT" table_index_value_bad.tl >/tmp/tealjit-table-index-value.out 2>&1; then
  echo "table_index_value_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "table value type mismatch" /tmp/tealjit-table-index-value.out

if "$LUAJIT" table_nested_value_bad.tl >/tmp/tealjit-table-nested-value.out 2>&1; then
  echo "table_nested_value_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "type mismatch" /tmp/tealjit-table-nested-value.out

if "$LUAJIT" for_ipairs_value_bad.tl >/tmp/tealjit-for-ipairs-value.out 2>&1; then
  echo "for_ipairs_value_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "type mismatch" /tmp/tealjit-for-ipairs-value.out

if "$LUAJIT" for_pairs_key_bad.tl >/tmp/tealjit-for-pairs-key.out 2>&1; then
  echo "for_pairs_key_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "type mismatch" /tmp/tealjit-for-pairs-key.out

if "$LUAJIT" for_pairs_value_bad.tl >/tmp/tealjit-for-pairs-value.out 2>&1; then
  echo "for_pairs_value_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "type mismatch" /tmp/tealjit-for-pairs-value.out

if "$LUAJIT" for_iterator_too_many_bad.tl >/tmp/tealjit-for-too-many.out 2>&1; then
  echo "for_iterator_too_many_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "too many variables for this iterator" /tmp/tealjit-for-too-many.out

if "$LUAJIT" for_inline_annotation_bad.tl >/tmp/tealjit-for-inline-annotation.out 2>&1; then
  echo "for_inline_annotation_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "expected" /tmp/tealjit-for-inline-annotation.out

if "$LUAJIT" for_ipairs_map_bad.tl >/tmp/tealjit-for-ipairs-map.out 2>&1; then
  echo "for_ipairs_map_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "attempting ipairs" /tmp/tealjit-for-ipairs-map.out

if "$LUAJIT" record_method_colon_arg_bad.tl >/tmp/tealjit-method-arg.out 2>&1; then
  echo "record_method_colon_arg_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "function argument type mismatch" /tmp/tealjit-method-arg.out

if "$LUAJIT" record_method_dot_missing_self_bad.tl >/tmp/tealjit-method-self.out 2>&1; then
  echo "record_method_dot_missing_self_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "function argument type mismatch" /tmp/tealjit-method-self.out

if "$LUAJIT" record_method_unknown_bad.tl >/tmp/tealjit-method-unknown.out 2>&1; then
  echo "record_method_unknown_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "unknown record field" /tmp/tealjit-method-unknown.out

if "$LUAJIT" record_method_self_unknown_bad.tl >/tmp/tealjit-method-self-unknown.out 2>&1; then
  echo "record_method_self_unknown_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "unknown record field" /tmp/tealjit-method-self-unknown.out
