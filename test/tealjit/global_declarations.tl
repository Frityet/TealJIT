global answer: integer = 42
local n: integer = answer
assert(n == 42)

global maybe: integer | nil
maybe = nil
assert(maybe == nil)

global function add(a: integer, b: integer): integer
  return a + b
end

local m: integer = add(20, 22)
assert(m == 42)

global hi = function(): string
  return "hi"
end

local greeting: string = hi()
assert(greeting == "hi")

global label = "world"
local copied_label: string = label
assert(copied_label == "world")

global record Export
  value: integer
  bump: function(Export, integer): integer
end

Export.value = 41

function Export:bump(delta: integer): integer
  return self.value + delta
end

assert(Export:bump(1) == 42)
assert(Export.bump(Export, 1) == 42)

global type Building

global record Person
  residence: Building
end

Person.residence = Export
assert(Person.residence.value == 41)
