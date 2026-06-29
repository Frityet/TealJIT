local record Point
  x: integer
  label: string
end

Point.x = 41
Point.label = "origin"

local n: integer = Point.x + 1
local s: string = Point.label

assert(n == 42)
assert(s == "origin")

local record export end

function export.add(a: integer, b: integer): integer
  return a + b
end

assert(export.add(2, 3) == 5)
