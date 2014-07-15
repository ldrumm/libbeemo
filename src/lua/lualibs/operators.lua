function intable(table, k)
    for _, v in ipairs(table) do
        if v == k then
            return true
        end
    end
    return false
end

function infix(op, fn)
    mt = {} 
    ops = {'__add', '__sub', '__mul', '__div', '__lt', '__gt', '__lte', '__gte', '__eq', '__call'}
    if not intable(ops, op) then error('unknown metatable:' .. op, 2) end
    mt[op] = fn
    return setmetatable({}, mt)
end

function insfix(op, fn)
    mt = {}
    mt[op] = function(self, var)
        print('infunction')
        if self.val > var then
            print('bigger')
            return self
        else
            print('smaller or equal')
            return var
        end
    end
    return setmetatable({}, mt)
end

function infix(f)
  local mt = { __sub = function(self, b) return f(self[1], b) end }
  return setmetatable({}, { __sub = function(a, _) return setmetatable({ a }, mt) end })
end

