local function keep(a: integer, label: string): integer
  return a
end

local f: function(integer, string): integer = keep
return f("bad", "label")
