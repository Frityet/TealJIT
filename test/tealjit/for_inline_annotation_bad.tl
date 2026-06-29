local flags: {string:integer} = { one = 1 }

for k: string, v: integer in pairs(flags) do
  print(k, v)
end
