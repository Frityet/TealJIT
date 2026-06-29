local record Ops
  mul: function(Ops, integer, integer): integer
end

function Ops:mul(a: integer, b: integer): integer
  return a * b
end

return Ops:mul("bad", 7)
