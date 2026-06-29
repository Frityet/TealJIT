local s: string = "teal"
local n: number = 2.0
local f: number = 2.5
local maybe: integer | nil = nil

assert(s is string)
assert(not (s is integer))
assert(n is integer)
assert(not (f is integer))
assert(maybe is nil)

maybe = 4
assert(maybe is integer)
assert(maybe is integer and maybe + 1 == 5)
assert(maybe is string or maybe == 4)

local ok: boolean = s is string
assert(ok)

local function is_string(x: string | nil): boolean
  return x is string
end

assert(is_string("x"))
assert(not is_string(nil))

if maybe is integer then
  assert(maybe + 10 == 14)
else
  error("integer check did not pass")
end
