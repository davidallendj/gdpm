#pragma once

namespace gdpm{
	/*
	Base class to prevent derived class from creating copies.
	*/
	class non_copyable{
	public:
		non_copyable(){}
	
	private:
		non_copyable(const non_copyable&);
		non_copyable& operator=(const non_copyable&);
	};

	/*
	Base class to prevent derived classes from moving objects.
	*/
	class non_movable{
		non_movable(const non_movable&) = delete;
		non_movable(non_movable&&) = delete;
	};
}
