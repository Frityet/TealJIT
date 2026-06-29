local text_or_int: string | integer = "ok"
local text_or_int_2: string | integer = 12
local wide: string | integer | boolean = text_or_int

assert(text_or_int == "ok")
assert(text_or_int_2 == 12)
assert(wide == "ok")

local numeric: integer | number = 12
local n: number = numeric
assert(n == 12)

local maybe_text: string | nil = nil
assert(maybe_text == nil)
maybe_text = "set"
assert(maybe_text == "set")

local values: {string | integer} = {"x", 7}
local v1: string | integer = values[1]
local v2: string | integer = values[2]
assert(v1 == "x")
assert(v2 == 7)

local by_name: {string: string | integer} = {a = "x", b = 1}
local named: string | integer = by_name["b"]
assert(named == 1)

local type TextOrInt = string | integer
local alias_value: TextOrInt = "alias"
assert(alias_value == "alias")

local function choose(flag: boolean, a: TextOrInt, b: TextOrInt): TextOrInt
   if flag then
      return a
   end
   return b
end

local chosen: TextOrInt = choose(false, "left", 9)
assert(chosen == 9)

local person: {string, integer} = {"Ada", 36}
local i: integer = 2
local field: string | integer = person[i]
assert(field == 36)

person[i] = 42
assert(person[2] == 42)
