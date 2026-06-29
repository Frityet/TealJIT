local flags: {string:integer} = { one = 1 }

for k, _ in next, flags, nil do
  local n: integer = k
end
