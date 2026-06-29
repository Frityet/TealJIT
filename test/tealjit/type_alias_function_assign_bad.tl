local type Callback = function(integer): string

local function bad(s: string): string
  return s
end

local cb: Callback = bad
return cb(1)
