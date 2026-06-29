local function takes_string(s: string): integer
  return 1
end

local f: function(integer): integer = takes_string
return f(1)
