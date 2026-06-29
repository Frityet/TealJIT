local function keep(a: integer, label: string): integer
  return a
end

local f: function(integer, string): integer = keep
local n: integer = f(42, "answer")
assert(n == 42)

local function join(name: string, suffix?: string): string
  return name .. (suffix or "")
end

local named: function(name: string, suffix?: string): string = join
local unnamed: function(string, ? string): string = join

assert(named("teal") == "teal")
assert(unnamed("teal", "jit") == "tealjit")

local record Ops
  cb: function(integer, string): integer
end

Ops.cb = keep
local m: integer = Ops.cb(7, "seven")
assert(m == 7)
