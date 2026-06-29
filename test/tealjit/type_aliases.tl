local type Count = integer
local type MaybeCount = Count | nil
local type Callback = function(Count, string): string

local c: Count = 42
local maybe: MaybeCount = nil

local function label(n: Count, text: string): string
  return text
end

local cb: Callback = label
assert(cb(c, "count") == "count")

local record Box
  value: Count
  cb: Callback
end

Box.value = c
Box.cb = label
assert(Box.cb(Box.value, "box") == "box")

local function callit(fn: Callback): string
  return fn(7, "param")
end

assert(callit(label) == "param")

do
  local type Count = string
  local inner: Count = "inner"
  assert(inner == "inner")
end

local after: Count = 9
assert(after == 9)
assert(maybe == nil)
