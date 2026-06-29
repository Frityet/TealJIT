local value: string | integer = 1

while value is integer do
   local n: integer = value
   assert(n == 1)
   break
end

local text: string | integer = "hello"

while not (text is integer) do
   local s: string = text
   assert(s == "hello")
   break
end
