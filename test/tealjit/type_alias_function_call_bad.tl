local type Callback = function(integer, string): string

local function label(n: integer, text: string): string
  return text
end

local cb: Callback = label
return cb("bad", "label")
