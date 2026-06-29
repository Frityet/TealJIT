local function opt(a: integer, label?: string): integer
  return a
end

local f: function(integer, ? string): integer = opt
return f()
