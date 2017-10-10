# move_only_std_function
A std::function implementation that is move only and does not require copy constructor of the functor
To this answer
https://stackoverflow.com/a/44442474/5371704

std::function can be move-constructed from rvalue of a functor object. And most implementations do that.

The "my target must be copy-constructable" requirement of std::function is due to its own requirement of being copy-constructable. std::function's type is defined only by its target's signature(eg: void(int)) and std::function itself is defined by the standard to be copy-constructable. So when you copy-construct a std::function, it needs to call the copy-ctor of the its target(the underlying functor). So it requires its target having one. It has no other choices.

Having the requirement that the target being copy-constructable, the standard does not say that the implementations should copy, instead of move, when you construct a std::function from a rvalue callable object. The implemention will probably only call the move-ctor of your callable object.

More detailed additional information with examples and tests:

For example in gcc(MSVC is similar) implementation for the ctor of std::function from any callable object:

template<typename _Res, typename... _ArgTypes>
  template<typename _Functor, typename>
    function<_Res(_ArgTypes...)>::
    function(_Functor __f)
    : _Function_base()
    {
        typedef _Function_handler<_Signature_type, _Functor> _My_handler;

        // don't need to care about details below, but when it uses __f, it 
        // either uses std::move, or passes it by references
        if (_My_handler::_M_not_empty_function(__f))
        {
           _My_handler::_M_init_functor(_M_functor, std::move(__f));
           _M_invoker = &_My_handler::_M_invoke;
           _M_manager = &_My_handler::_M_manager;
        }
    }

passing by value of the argument of "_Functor __f" will use its move constructor if it has one, and it will use its copy constructor if it does not have a move ctor. As the following test program can demonstrate:

int main(){
    using namespace std;
    struct TFunctor
    {
        TFunctor() = default;
        TFunctor(const TFunctor&) { cout << "cp ctor called" << endl; }
        TFunctor(TFunctor&&) { cout << "mv ctor called" << endl; };
        void operator()(){}
    };

    {   //!!!!COPY CTOR of TFunctor is NEVER called in this scope
        TFunctor myFunctor;

        //TFunctor move ctor called here
        function<void()> myStdFuncTemp{ std::move(myFunctor) }; 

        function<void()> myStdFunc{ move(myStdFuncTemp) }; 
    }

    {   //TFunctor copy ctor is called twice in this scope
        TFunctor myFunctor;

        //TFunctor copy ctor called once here
        function<void()> myStdFuncTemp{ myFunctor };

        //TFunctor copy ctor called once here
        function<void()> myStdFunc{ myStdFuncTemp };
    }
}

Finally, you could make a unstd::function_only_movable which has almost everything the same with std::function but deletes its own copy ctor so it does not need to require the target callable object to have one copy ctor. You also need to only construct it from rvalue of callable objects.
