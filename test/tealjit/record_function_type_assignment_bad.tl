local record Ops
  cb: function(integer): integer
end

local function takes_string(s: string): integer
  return 1
end

Ops.cb = takes_string
