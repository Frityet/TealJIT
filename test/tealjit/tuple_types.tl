local person: {string, integer} = {"Ada", 36}
local name: string = person[1]
local age: integer = person[2]

assert(name == "Ada")
assert(age == 36)

local short: {number, string, boolean} = {1, "x"}
local first: number = short[1]
local second: string = short[2]

assert(first == 1)
assert(second == "x")

local keyed: {string, integer} = {[1] = "x", [2] = 7}
local keyed_name: string = keyed[1]
local keyed_age: integer = keyed[2]

assert(keyed_name == "x")
assert(keyed_age == 7)

local function get_age(p: {string, integer}): integer
   return p[2]
end

assert(get_age({"Grace", 42}) == 42)

local homogeneous: {number, number} = {1, 2}
local h: number = homogeneous[2]
assert(h == 2)

local i: integer = 2
local hv: number = homogeneous[i]
assert(hv == 2)
homogeneous[i] = 5
assert(homogeneous[2] == 5)
