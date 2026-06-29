local text_or_int: string | integer = "hello"
local is_text: boolean = text_or_int is string
local is_int: boolean = text_or_int is integer
local is_text_or_int: boolean = text_or_int is string | integer

assert(is_text)
assert(not is_int)
assert(is_text_or_int)

local other: string | integer = 12
assert(other is integer)
assert(not (other is string))
assert(other is string | integer)
assert(not (other is string | boolean))

local concrete = "plain"
assert(concrete is string | integer)
assert(not (concrete is integer | boolean))
