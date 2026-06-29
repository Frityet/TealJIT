local value: string | integer = 12

if not (value is string) then
   local text: string = value
   return text
end

return ""
