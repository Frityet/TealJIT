local type Count = integer
local type MaybeCount = Count | nil

local maybe: MaybeCount = nil
local c: Count = maybe
return c
