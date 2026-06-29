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

if text_or_int is string then
   local narrowed_text: string = text_or_int
   assert(narrowed_text == "hello")
end

if other is integer then
   local narrowed_int: integer = other
   assert(narrowed_int == 12)
end

local maybe_number: integer | nil = 4
if maybe_number is integer then
   local narrowed_number: integer = maybe_number
   assert(narrowed_number == 4)
end
