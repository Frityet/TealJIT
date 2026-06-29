local function add(a: integer, b: integer): integer
  return a + b
end

local function caller(): integer
  return add("bad", 1)
end

return caller()
