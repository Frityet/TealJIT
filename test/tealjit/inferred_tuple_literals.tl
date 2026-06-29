local person = {"Ada", 36}
local name: string = person[1]
local age: integer = person[2]

assert(name == "Ada")
assert(age == 36)

local keyed = {[2] = 7, [1] = "x"}
local keyed_name: string = keyed[1]
local keyed_age: integer = keyed[2]

assert(keyed_name == "x")
assert(keyed_age == 7)

local xs = {1, 2}
local first: integer = xs[1]
xs[2] = 3

assert(first == 1)
assert(xs[2] == 3)
