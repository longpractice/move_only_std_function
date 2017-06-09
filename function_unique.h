/*
 * unique_function.h
 *
 *  Created on: Jun 9, 2017
 *      Author: Mr. X
 */

#ifndef UNIQUE_FUNCTION_H_
#define UNIQUE_FUNCTION_H_

#pragma GCC system_header

#include <bits/c++config.h>
#include <bits/stl_function.h>

#if __cplusplus >= 201103L

#include <typeinfo>
#include <new>
#include <tuple>
#include <type_traits>
#include <bits/functexcept.h>
#include <bits/functional_hash.h>
#include <functional>

namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

  template<typename _Signature>
    class unique_function;

  /// Base class of all polymorphic function object wrappers.
  class _Unique_Function_base
  {
  public:
    static const std::size_t _M_max_size = sizeof(_Nocopy_types);
    static const std::size_t _M_max_align = __alignof__(_Nocopy_types);

    template<typename _Functor>
      class _Base_manager
      {
      protected:
	static const bool __stored_locally =
	(__is_location_invariant<_Functor>::value
	 && sizeof(_Functor) <= _M_max_size
	 && __alignof__(_Functor) <= _M_max_align
	 && (_M_max_align % __alignof__(_Functor) == 0));

	typedef integral_constant<bool, __stored_locally> _Local_storage;

	// Retrieve a pointer to the function object
	static _Functor*
	_M_get_pointer(const _Any_data& __source)
	{
	  const _Functor* __ptr =
	    __stored_locally? std::__addressof(__source._M_access<_Functor>())
	    /* have stored a pointer */ : __source._M_access<_Functor*>();
	  return const_cast<_Functor*>(__ptr);
	}

	// Clone a location-invariant function object that fits within
	// an _Any_data structure.
	static void
	_M_clone(_Any_data& __dest, const _Any_data& __source, true_type)
	{
	  new (__dest._M_access()) _Functor(__source._M_access<_Functor>());
	}

	// Clone a function object that is not location-invariant or
	// that cannot fit into an _Any_data structure.
	static void
	_M_clone(_Any_data& __dest, const _Any_data& __source, false_type)
	{
	  __dest._M_access<_Functor*>() =
	    new _Functor(*__source._M_access<_Functor*>());
	}

	// Destroying a location-invariant object may still require
	// destruction.
	static void
	_M_destroy(_Any_data& __victim, true_type)
	{
	  __victim._M_access<_Functor>().~_Functor();
	}

	// Destroying an object located on the heap.
	static void
	_M_destroy(_Any_data& __victim, false_type)
	{
	  delete __victim._M_access<_Functor*>();
	}

      public:
	static bool
	_M_manager(_Any_data& __dest, const _Any_data& __source,
		   _Manager_operation __op)
	{
	  switch (__op)
	    {
#ifdef __GXX_RTTI
	    case __get_type_info:
	      __dest._M_access<const type_info*>() = &typeid(_Functor);
	      break;
#endif
	    case __get_functor_ptr:
	      __dest._M_access<_Functor*>() = _M_get_pointer(__source);
	      break;

	    case __clone_functor:
	      _M_clone(__dest, __source, _Local_storage());
	      break;

	    case __destroy_functor:
	      _M_destroy(__dest, _Local_storage());
	      break;
	    }
	  return false;
	}

	static void
	_M_init_functor(_Any_data& __functor, _Functor&& __f)
	{ _M_init_functor(__functor, std::move(__f), _Local_storage()); }

	template<typename _Signature>
	  static bool
	  _M_not_empty_function(const unique_function<_Signature>& __f)
	  { return static_cast<bool>(__f); }

	template<typename _Tp>
	  static bool
	  _M_not_empty_function(_Tp* const& __fp)
	  { return __fp; }

	template<typename _Class, typename _Tp>
	  static bool
	  _M_not_empty_function(_Tp _Class::* const& __mp)
	  { return __mp; }

	template<typename _Tp>
	  static bool
	  _M_not_empty_function(const _Tp&)
	  { return true; }

      private:
	static void
	_M_init_functor(_Any_data& __functor, _Functor&& __f, true_type)
	{ new (__functor._M_access()) _Functor(std::move(__f)); }

	static void
	_M_init_functor(_Any_data& __functor, _Functor&& __f, false_type)
	{ __functor._M_access<_Functor*>() = new _Functor(std::move(__f)); }
      };

    template<typename _Functor>
      class _Ref_manager : public _Base_manager<_Functor*>
      {
	typedef _Unique_Function_base::_Base_manager<_Functor*> _Base;

      public:
	static bool
	_M_manager(_Any_data& __dest, const _Any_data& __source,
		   _Manager_operation __op)
	{
	  switch (__op)
	    {
#ifdef __GXX_RTTI
	    case __get_type_info:
	      __dest._M_access<const type_info*>() = &typeid(_Functor);
	      break;
#endif
	    case __get_functor_ptr:
	      __dest._M_access<_Functor*>() = *_Base::_M_get_pointer(__source);
	      return is_const<_Functor>::value;
	      break;

	    default:
	      _Base::_M_manager(__dest, __source, __op);
	    }
	  return false;
	}

	static void
	_M_init_functor(_Any_data& __functor, reference_wrapper<_Functor> __f)
	{
	  _Base::_M_init_functor(__functor, std::__addressof(__f.get()));
	}
      };

    _Unique_Function_base() : _M_manager(0) { }

    ~_Unique_Function_base()
    {
      if (_M_manager)
	_M_manager(_M_functor, _M_functor, __destroy_functor);
    }


    bool _M_empty() const { return !_M_manager; }

    typedef bool (*_Manager_type)(_Any_data&, const _Any_data&,
				  _Manager_operation);

    _Any_data     _M_functor;
    _Manager_type _M_manager;
  };

  template<typename _Signature, typename _Functor>
    class _Unique_Function_handler;

  template<typename _Res, typename _Functor, typename... _ArgTypes>
    class _Unique_Function_handler<_Res(_ArgTypes...), _Functor>
    : public _Unique_Function_base::_Base_manager<_Functor>
    {
      typedef _Unique_Function_base::_Base_manager<_Functor> _Base;

    public:
      static _Res
      _M_invoke(const _Any_data& __functor, _ArgTypes... __args)
      {
	return (*_Base::_M_get_pointer(__functor))(
	    std::forward<_ArgTypes>(__args)...);
      }
    };

  template<typename _Functor, typename... _ArgTypes>
    class _Unique_Function_handler<void(_ArgTypes...), _Functor>
    : public _Unique_Function_base::_Base_manager<_Functor>
    {
      typedef _Unique_Function_base::_Base_manager<_Functor> _Base;

     public:
      static void
      _M_invoke(const _Any_data& __functor, _ArgTypes... __args)
      {
	(*_Base::_M_get_pointer(__functor))(
	    std::forward<_ArgTypes>(__args)...);
      }
    };

  template<typename _Res, typename _Functor, typename... _ArgTypes>
    class _Unique_Function_handler<_Res(_ArgTypes...), reference_wrapper<_Functor> >
    : public _Unique_Function_base::_Ref_manager<_Functor>
    {
      typedef _Unique_Function_base::_Ref_manager<_Functor> _Base;

     public:
      static _Res
      _M_invoke(const _Any_data& __functor, _ArgTypes... __args)
      {
	return __callable_functor(**_Base::_M_get_pointer(__functor))(
	      std::forward<_ArgTypes>(__args)...);
      }
    };

  template<typename _Functor, typename... _ArgTypes>
    class _Unique_Function_handler<void(_ArgTypes...), reference_wrapper<_Functor> >
    : public _Unique_Function_base::_Ref_manager<_Functor>
    {
      typedef _Unique_Function_base::_Ref_manager<_Functor> _Base;

     public:
      static void
      _M_invoke(const _Any_data& __functor, _ArgTypes... __args)
      {
	__callable_functor(**_Base::_M_get_pointer(__functor))(
	    std::forward<_ArgTypes>(__args)...);
      }
    };

  template<typename _Class, typename _Member, typename _Res,
	   typename... _ArgTypes>
    class _Unique_Function_handler<_Res(_ArgTypes...), _Member _Class::*>
    : public _Unique_Function_handler<void(_ArgTypes...), _Member _Class::*>
    {
      typedef _Unique_Function_handler<void(_ArgTypes...), _Member _Class::*>
	_Base;

     public:
      static _Res
      _M_invoke(const _Any_data& __functor, _ArgTypes... __args)
      {
	return std::mem_fn(_Base::_M_get_pointer(__functor)->__value)(
	    std::forward<_ArgTypes>(__args)...);
      }
    };

  template<typename _Class, typename _Member, typename... _ArgTypes>
    class _Unique_Function_handler<void(_ArgTypes...), _Member _Class::*>
    : public _Unique_Function_base::_Base_manager<
		 _Simple_type_wrapper< _Member _Class::* > >
    {
      typedef _Member _Class::* _Functor;
      typedef _Simple_type_wrapper<_Functor> _Wrapper;
      typedef _Unique_Function_base::_Base_manager<_Wrapper> _Base;

    public:
      static bool
      _M_manager(_Any_data& __dest, const _Any_data& __source,
		 _Manager_operation __op)
      {
	switch (__op)
	  {
#ifdef __GXX_RTTI
	  case __get_type_info:
	    __dest._M_access<const type_info*>() = &typeid(_Functor);
	    break;
#endif
	  case __get_functor_ptr:
	    __dest._M_access<_Functor*>() =
	      &_Base::_M_get_pointer(__source)->__value;
	    break;

	  default:
	    _Base::_M_manager(__dest, __source, __op);
	  }
	return false;
      }

      static void
      _M_invoke(const _Any_data& __functor, _ArgTypes... __args)
      {
	std::mem_fn(_Base::_M_get_pointer(__functor)->__value)(
	    std::forward<_ArgTypes>(__args)...);
      }
    };

  template<typename _From, typename _To>
    using __check_func_return_type
      = __or_<is_void<_To>, is_convertible<_From, _To>>;

  /**
   *  @brief Primary class template for std::unique_function.
   *  @ingroup functors
   *
   *  Polymorphic function wrapper.
   */
  template<typename _Res, typename... _ArgTypes>
    class unique_function<_Res(_ArgTypes...)>
    : public _Maybe_unary_or_binary_function<_Res, _ArgTypes...>,
      private _Unique_Function_base
    {
      typedef _Res _Signature_type(_ArgTypes...);

      template<typename _Functor>
	using _Invoke = decltype(__callable_functor(std::declval<_Functor&>())
				 (std::declval<_ArgTypes>()...) );

      // Used so the return type convertibility checks aren't done when
      // performing overload resolution for copy construction/assignment.
      template<typename _Tp>
	using _NotSelf = __not_<is_same<_Tp, unique_function>>;

      template<typename _Functor>
	using _Callable
	  = __and_<_NotSelf<_Functor>,
		   __check_func_return_type<_Invoke<_Functor>, _Res>>;

      template<typename _Cond, typename _Tp>
	using _Requires = typename enable_if<_Cond::value, _Tp>::type;

    public:
      typedef _Res result_type;

      // [3.7.2.1] construct/copy/destroy

      /**
       *  @brief Default construct creates an empty function call wrapper.
       *  @post @c !(bool)*this
       */
      unique_function() noexcept
      : _Unique_Function_base() { }

      /**
       *  @brief Creates an empty function call wrapper.
       *  @post @c !(bool)*this
       */
      unique_function(nullptr_t) noexcept
      : _Unique_Function_base() { }

      /**
       *  @brief %Function copy constructor.
       *  @param __x A %function object with identical call signature.
       *  @post @c bool(*this) == bool(__x)
       *
       *  The newly-created %function contains a copy of the target of @a
       *  __x (if it has one).
       */
      unique_function(const unique_function& __x);

      /**
       *  @brief %Function move constructor.
       *  @param __x A %function object rvalue with identical call signature.
       *
       *  The newly-created %function contains the target of @a __x
       *  (if it has one).
       */
      unique_function(unique_function&& __x) : _Unique_Function_base()
      {
	__x.swap(*this);
      }

      // TODO: needs allocator_arg_t

      /**
       *  @brief Builds a %function that targets a copy of the incoming
       *  function object.
       *  @param __f A %function object that is callable with parameters of
       *  type @c T1, @c T2, ..., @c TN and returns a value convertible
       *  to @c Res.
       *
       *  The newly-created %function object will target a copy of
       *  @a __f. If @a __f is @c reference_wrapper<F>, then this function
       *  object will contain a reference to the function object @c
       *  __f.get(). If @a __f is a NULL function pointer or NULL
       *  pointer-to-member, the newly-created object will be empty.
       *
       *  If @a __f is a non-NULL function pointer or an object of type @c
       *  reference_wrapper<F>, this function will not throw.
       */
      template<typename _Functor,
	       typename = _Requires<_Callable<_Functor>, void>>
	unique_function(_Functor);

      /**
       *  @brief %Function assignment operator.
       *  @param __x A %function with identical call signature.
       *  @post @c (bool)*this == (bool)x
       *  @returns @c *this
       *
       *  The target of @a __x is copied to @c *this. If @a __x has no
       *  target, then @c *this will be empty.
       *
       *  If @a __x targets a function pointer or a reference to a function
       *  object, then this operation will not throw an %exception.
       */
      unique_function&
      operator=(const unique_function& __x)
      {
	unique_function(__x).swap(*this);
	return *this;
      }

      /**
       *  @brief %Function move-assignment operator.
       *  @param __x A %function rvalue with identical call signature.
       *  @returns @c *this
       *
       *  The target of @a __x is moved to @c *this. If @a __x has no
       *  target, then @c *this will be empty.
       *
       *  If @a __x targets a function pointer or a reference to a function
       *  object, then this operation will not throw an %exception.
       */
      unique_function&
      operator=(unique_function&& __x)
      {
	unique_function(std::move(__x)).swap(*this);
	return *this;
      }

      /**
       *  @brief %Function assignment to zero.
       *  @post @c !(bool)*this
       *  @returns @c *this
       *
       *  The target of @c *this is deallocated, leaving it empty.
       */
      unique_function&
      operator=(nullptr_t)
      {
	if (_M_manager)
	  {
	    _M_manager(_M_functor, _M_functor, __destroy_functor);
	    _M_manager = 0;
	    _M_invoker = 0;
	  }
	return *this;
      }

      /**
       *  @brief %Function assignment to a new target.
       *  @param __f A %function object that is callable with parameters of
       *  type @c T1, @c T2, ..., @c TN and returns a value convertible
       *  to @c Res.
       *  @return @c *this
       *
       *  This  %function object wrapper will target a copy of @a
       *  __f. If @a __f is @c reference_wrapper<F>, then this function
       *  object will contain a reference to the function object @c
       *  __f.get(). If @a __f is a NULL function pointer or NULL
       *  pointer-to-member, @c this object will be empty.
       *
       *  If @a __f is a non-NULL function pointer or an object of type @c
       *  reference_wrapper<F>, this function will not throw.
       */
      template<typename _Functor>
	_Requires<_Callable<typename decay<_Functor>::type>, unique_function&>
	operator=(_Functor&& __f)
	{
	  unique_function(std::forward<_Functor>(__f)).swap(*this);
	  return *this;
	}

      /// @overload
      template<typename _Functor>
	unique_function&
	operator=(reference_wrapper<_Functor> __f) noexcept
	{
	  unique_function(__f).swap(*this);
	  return *this;
	}

      // [3.7.2.2] function modifiers

      /**
       *  @brief Swap the targets of two %function objects.
       *  @param __x A %function with identical call signature.
       *
       *  Swap the targets of @c this function object and @a __f. This
       *  function will not throw an %exception.
       */
      void swap(unique_function& __x)
      {
	std::swap(_M_functor, __x._M_functor);
	std::swap(_M_manager, __x._M_manager);
	std::swap(_M_invoker, __x._M_invoker);
      }

      // TODO: needs allocator_arg_t
      /*
      template<typename _Functor, typename _Alloc>
	void
	assign(_Functor&& __f, const _Alloc& __a)
	{
	  function(allocator_arg, __a,
		   std::forward<_Functor>(__f)).swap(*this);
	}
      */

      // [3.7.2.3] function capacity

      /**
       *  @brief Determine if the %function wrapper has a target.
       *
       *  @return @c true when this %function object contains a target,
       *  or @c false when it is empty.
       *
       *  This function will not throw an %exception.
       */
      explicit operator bool() const noexcept
      { return !_M_empty(); }

      // [3.7.2.4] function invocation

      /**
       *  @brief Invokes the function targeted by @c *this.
       *  @returns the result of the target.
       *  @throws bad_function_call when @c !(bool)*this
       *
       *  The function call operator invokes the target function object
       *  stored by @c this.
       */
      _Res operator()(_ArgTypes... __args) const;

#ifdef __GXX_RTTI
      // [3.7.2.5] function target access
      /**
       *  @brief Determine the type of the target of this function object
       *  wrapper.
       *
       *  @returns the type identifier of the target function object, or
       *  @c typeid(void) if @c !(bool)*this.
       *
       *  This function will not throw an %exception.
       */
      const type_info& target_type() const noexcept;

      /**
       *  @brief Access the stored target function object.
       *
       *  @return Returns a pointer to the stored target function object,
       *  if @c typeid(Functor).equals(target_type()); otherwise, a NULL
       *  pointer.
       *
       * This function will not throw an %exception.
       */
      template<typename _Functor>       _Functor* target() noexcept;

      /// @overload
      template<typename _Functor> const _Functor* target() const noexcept;
#endif

    private:
      typedef _Res (*_Invoker_type)(const _Any_data&, _ArgTypes...);
      _Invoker_type _M_invoker;
  };

  // Out-of-line member definitions.
  template<typename _Res, typename... _ArgTypes>
    unique_function<_Res(_ArgTypes...)>::
    unique_function(const unique_function& __x)
    : _Unique_Function_base()
    {
      if (static_cast<bool>(__x))
	{
	  _M_invoker = __x._M_invoker;
	  _M_manager = __x._M_manager;
	  __x._M_manager(_M_functor, __x._M_functor, __clone_functor);
	}
    }

  template<typename _Res, typename... _ArgTypes>
    template<typename _Functor, typename>
      unique_function<_Res(_ArgTypes...)>::
      unique_function(_Functor __f)
      : _Unique_Function_base()
      {
	typedef _Unique_Function_handler<_Signature_type, _Functor> _My_handler;

	if (_My_handler::_M_not_empty_function(__f))
	  {
	    _My_handler::_M_init_functor(_M_functor, std::move(__f));
	    _M_invoker = &_My_handler::_M_invoke;
	    _M_manager = &_My_handler::_M_manager;
	  }
      }

  template<typename _Res, typename... _ArgTypes>
    _Res
    unique_function<_Res(_ArgTypes...)>::
    operator()(_ArgTypes... __args) const
    {
      if (_M_empty())
	__throw_bad_function_call();
      return _M_invoker(_M_functor, std::forward<_ArgTypes>(__args)...);
    }

#ifdef __GXX_RTTI
  template<typename _Res, typename... _ArgTypes>
    const type_info&
    unique_function<_Res(_ArgTypes...)>::
    target_type() const noexcept
    {
      if (_M_manager)
	{
	  _Any_data __typeinfo_result;
	  _M_manager(__typeinfo_result, _M_functor, __get_type_info);
	  return *__typeinfo_result._M_access<const type_info*>();
	}
      else
	return typeid(void);
    }

  template<typename _Res, typename... _ArgTypes>
    template<typename _Functor>
      _Functor*
      unique_function<_Res(_ArgTypes...)>::
      target() noexcept
      {
	if (typeid(_Functor) == target_type() && _M_manager)
	  {
	    _Any_data __ptr;
	    if (_M_manager(__ptr, _M_functor, __get_functor_ptr)
		&& !is_const<_Functor>::value)
	      return 0;
	    else
	      return __ptr._M_access<_Functor*>();
	  }
	else
	  return 0;
      }

  template<typename _Res, typename... _ArgTypes>
    template<typename _Functor>
      const _Functor*
      unique_function<_Res(_ArgTypes...)>::
      target() const noexcept
      {
	if (typeid(_Functor) == target_type() && _M_manager)
	  {
	    _Any_data __ptr;
	    _M_manager(__ptr, _M_functor, __get_functor_ptr);
	    return __ptr._M_access<const _Functor*>();
	  }
	else
	  return 0;
      }
#endif

  // [20.7.15.2.6] null pointer comparisons

  /**
   *  @brief Compares a polymorphic function object wrapper against 0
   *  (the NULL pointer).
   *  @returns @c true if the wrapper has no target, @c false otherwise
   *
   *  This function will not throw an %exception.
   */
  template<typename _Res, typename... _Args>
    inline bool
    operator==(const unique_function<_Res(_Args...)>& __f, nullptr_t) noexcept
    { return !static_cast<bool>(__f); }

  /// @overload
  template<typename _Res, typename... _Args>
    inline bool
    operator==(nullptr_t, const unique_function<_Res(_Args...)>& __f) noexcept
    { return !static_cast<bool>(__f); }

  /**
   *  @brief Compares a polymorphic function object wrapper against 0
   *  (the NULL pointer).
   *  @returns @c false if the wrapper has no target, @c true otherwise
   *
   *  This function will not throw an %exception.
   */
  template<typename _Res, typename... _Args>
    inline bool
    operator!=(const unique_function<_Res(_Args...)>& __f, nullptr_t) noexcept
    { return static_cast<bool>(__f); }

  /// @overload
  template<typename _Res, typename... _Args>
    inline bool
    operator!=(nullptr_t, const unique_function<_Res(_Args...)>& __f) noexcept
    { return static_cast<bool>(__f); }

  // [20.7.15.2.7] specialized algorithms

  /**
   *  @brief Swap the targets of two polymorphic function object wrappers.
   *
   *  This function will not throw an %exception.
   */
  template<typename _Res, typename... _Args>
    inline void
    swap(unique_function<_Res(_Args...)>& __x, unique_function<_Res(_Args...)>& __y)
    { __x.swap(__y); }

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace std

#endif // C++11




#endif /* UNIQUE_FUNCTION_H_ */
