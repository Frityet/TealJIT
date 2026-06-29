local record Ops
  value: integer
  mul: function(Ops, integer, integer): integer
  get: function(Ops): integer
end

Ops.value = 42

function Ops:mul(a: integer, b: integer): integer
  return a * b
end

function Ops:get(): integer
  return self.value
end

assert(Ops:mul(6, 7) == 42)
assert(Ops.mul(Ops, 6, 7) == 42)
assert(Ops:get() == 42)
