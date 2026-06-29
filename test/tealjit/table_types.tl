local xs: {integer} = {1, 2, 3}
local n: integer = xs[1]
assert(n == 1)
assert(#xs == 3)

xs[2] = 42
assert(xs[2] == 42)

local flags: {string:boolean} = { ready = true, done = false }
local ready: boolean = flags["ready"]
assert(ready == true)

flags.done = true
assert(flags.done == true)

local nested: {string:{integer}} = { a = {4, 5} }
local inner: integer = nested["a"][2]
assert(inner == 5)

local inferred = {}
inferred[1] = 10
local inferred_value: integer = inferred[1]
assert(inferred_value == 10)

global table_global: {string:boolean} = {}
table_global["ok"] = true
assert(table_global.ok == true)
