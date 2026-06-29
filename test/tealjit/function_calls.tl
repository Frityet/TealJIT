local function add(a: integer, b: integer): integer
  return a + b
end

local n: integer = add(20, 22)
assert(n == 42)

local function greet(name: string, suffix?: string): string
  return name .. (suffix or "")
end

assert(greet("teal") == "teal")
assert(greet("teal", "jit") == "tealjit")

local record export end

function export.mul(a: integer, b: integer): integer
  return a * b
end

local m: integer = export.mul(6, 7)
assert(m == 42)
