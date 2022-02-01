local _class={}
 
function class(super)
    -- 构造一个类模版，并且包含一个vtable
    -- 把vtable保存在全局的_class里，并生成一个new函数
    -- new函数返回一个对象，该对象依次调用构造函数，并把元表设置为该类模版的vtable
	local class_type={}
	class_type.ctor=false
	class_type.super=super --指向父类
	class_type.new=function(...) 
			local obj={}
			do
				local create
				create = function(c,...)
					if c.super then
						create(c.super,...) --往上到最父类,从最父类开始构造
					end
					if c.ctor then
						c.ctor(obj,...) -- 仿造C++惯例，依次从父到子调用ctor,对obj表构造
					end
				end
 
				create(class_type,...)
			end
            --obj的方法其实是空的，对obj的方法调用都会查class[class_type]
            --比如 a = test.new(1), 对a对象的方法查询会去查test的方法，从而实现了对象和类模版的绑定
			setmetatable(obj,{ __index=_class[class_type] })
			return obj
		end
	local vtbl={}
	_class[class_type]=vtbl
 
    -- obj.b = xx,会把xx存放在class[class_type]里，也就是续表
	setmetatable(class_type,{__newindex=
		function(t,k,v)
			vtbl[k]=v
		end
	})
 
	if super then
        -- 查询方法的时候如果有父类，把父类的方法保存在自己的vtable里
        -- 以后调用就不用查父类了
		setmetatable(vtbl,{__index=
			function(t,k)
				local ret=_class[super][k]
				vtbl[k]=ret
				return ret
			end
		})
	end
 
	return class_type
end

baseType = class()
function baseType:ctor(x)
    print("baseType ctor")
    self.x = x
end

function baseType:print_x()
    print(self.x)
end

function baseType:hello()
    print("hello basetype")
end

test = class(baseType)
function test:ctor()
    print("test ctor")
end

function test:hello()
    print("test hello")
end


a = test.new(234)
a:print_x()
a.hello()
a:hello()

baseType.y = 100
print(a.y)
