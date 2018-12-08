///
#ifndef PRIVEXEC_COMUTILS_HPP
#define PRIVEXEC_COMUTILS_HPP
#include <comdef.h>
#include <Sddl.h>
#include <Shlwapi.h>

namespace priv {
template <class T> class comptr {
public:
  comptr() { ptr = NULL; }
  comptr(T *p) {
    ptr = p;
    if (ptr != NULL)
      ptr->AddRef();
  }
  comptr(const comptr<T> &sptr) {
    ptr = sptr.ptr;
    if (ptr != NULL)
      ptr->AddRef();
  }
  T **operator&() { return &ptr; }
  T *operator->() { return ptr; }
  T *operator=(T *p) {
    if (*this != p) {
      ptr = p;
      if (ptr != NULL)
        ptr->AddRef();
    }
    return *this;
  }
  operator T *() const { return ptr; }
  template <class I> HRESULT QueryInterface(REFCLSID rclsid, I **pp) {
    if (pp != NULL) {
      return ptr->QueryInterface(rclsid, (void **)pp);
    } else {
      return E_FAIL;
    }
  }
  HRESULT CoCreateInstance(REFCLSID clsid, IUnknown *pUnknown,
                           REFIID interfaceId,
                           DWORD dwClsContext = CLSCTX_ALL) {
    HRESULT hr = ::CoCreateInstance(clsid, pUnknown, dwClsContext, interfaceId,
                                    (void **)&ptr);
    return hr;
  }
  ~comptr() {
    if (ptr != NULL)
      ptr->Release();
  }

private:
  T *ptr;
};
} // namespace priv

#endif