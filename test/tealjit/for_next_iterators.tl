local flags: {string:integer} = { one = 1, two = 2 }
local total = 0

for k, v in next, flags, nil do
  local key: string = k
  local value: integer = v
  total = total + #key + value
end

assert(total > 0)

local xs: {boolean} = {true, false}
local count = 0

for i, v in next, xs, nil do
  local idx: integer = i
  local value: boolean = v
  count = count + idx
  assert(value == true or value == false)
end

assert(count > 0)
