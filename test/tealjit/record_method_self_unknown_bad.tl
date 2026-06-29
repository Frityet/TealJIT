local record Ops
  value: integer
  get: function(Ops): integer
end

function Ops:get(): integer
  return self.missing
end

return Ops:get()
