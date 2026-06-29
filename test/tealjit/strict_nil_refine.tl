local maybe_int: integer | nil = 4
local asserted: integer = assert(maybe_int)
assert(asserted == 4)

local missing_int: integer | nil = nil
local fallback_int: integer = missing_int or 7
assert(fallback_int == 7)

local maybe_text: string | nil = "hello"
local fallback_text: string = maybe_text or "fallback"
assert(fallback_text == "hello")
