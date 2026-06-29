local xs: {string} = {"one", "two"}
local seen = 0

for i, v in ipairs(xs) do
  local idx: integer = i
  local item: string = v
  seen = seen + idx + #item
end

assert(seen > 0)

local flags: {string:integer} = { one = 1, two = 2 }
local total = 0

for k, v in pairs(flags) do
  local key: string = k
  local value: integer = v
  total = total + #key + value
end

assert(total > 0)

local nested: {string:{integer}} = { a = {3, 4} }

for _, values in pairs(nested) do
  for i, value in ipairs(values) do
    local idx: integer = i
    local n: integer = value
    assert(idx >= 1)
    assert(n >= 3)
  end
end
