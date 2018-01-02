#include "python/bindings/pymodule/tpl/cpp_tpl_types.h"

#include <pybind11/eval.h>

#include "cpp/name_trait.h"


TypeRegistry::TypeRegistry() {
  // Import modules into `locals_`.
  globals_ = py::globals();
  exec(R"""(
import numpy as np; import ctypes

def _get_type_name(t):
  # Gets scoped type name as a string.
  prefix = t.__module__ + "."
  if prefix == "__builtin__.":
    prefix = ""
  return prefix + t.__name__
)""");

  RegisterCommon();
}

const TypeRegistry& TypeRegistry::GetPyInstance() {
  auto tr_module = py::module::import("pymodule.tpl.cpp_tpl_types");
  py::object type_registry_py = tr_module.attr("type_registry");
  const TypeRegistry* type_registry =
      py::cast<const TypeRegistry*>(type_registry_py);
  return *type_registry;
}

py::handle TypeRegistry::DoGetPyType(const std::type_info& tinfo) const {
  // Check if it's a custom-registered type.
  size_t cpp_key = std::type_index(tinfo).hash_code();
  auto iter = cpp_to_py_.find(cpp_key);
  if (iter != cpp_to_py_.end()) {
    return iter->second;
  } else {
    // Get from pybind11-registered types.
    // WARNING: Internal API :(
    auto* info = py::detail::get_type_info(tinfo);
    if (!info) {
      throw std::runtime_error("Unknown type!");
    }
    return py::handle(reinterpret_cast<PyObject*>(info->type));
  }
}

py::handle TypeRegistry::GetPyTypeCanonical(py::handle py_type) const {
  // Since there's no easy / good way to expose C++ type id's to Python,
  // just canonicalize Python types.
  return py_to_py_canonical_.attr("get")(py_type, py_type);
}

py::tuple TypeRegistry::GetPyTypesCanonical(py::tuple py_types) const {
  py::tuple out(py_types.size());
  for (int i = 0; i < py_types.size(); ++i) {
    out[i] = GetPyTypeCanonical(py_types[i]);
  }
  return out;
}

py::str TypeRegistry::GetName(py::handle py_type) const {
  py::handle py_type_fin = GetPyTypeCanonical(py_type);
  py::object out = py_name_.attr("get")(py_type_fin);
  if (out.is(py::none())) {
    out = eval("_get_type_name")(py_type_fin);
  }
  return out;
}

py::tuple TypeRegistry::GetNames(py::tuple py_types) const {
  py::tuple out(py_types.size());
  for (int i = 0; i < py_types.size(); ++i) {
    out[i] = GetName(py_types[i]);
  }
  return out;
}

void TypeRegistry::Register(
      size_t cpp_key, py::tuple py_types, const std::string& name) {
  // TODO: Ensure no duplicate `py_types` or `cpp_types`?
  // Make a `unique_dict` that throws if overlap occurs?
  py::handle py_canonical = py_types[0];
  cpp_to_py_[cpp_key] = py_canonical;
  for (auto py_type : py_types) {
    py_to_py_canonical_[py_type] = py_canonical;
  }
  py_name_[py_canonical] = name;
}

template <typename T>
void TypeRegistry::RegisterType(
    py::tuple py_types, const std::string& name_override) {
  size_t cpp_key = std::type_index(typeid(T)).hash_code();
  std::string name = name_override;
  if (name.empty()) {
    name = py::cast<std::string>(eval("_get_type_name")(py_types[0]));
  }
  Register(cpp_key, py_types, name);
}

struct TypeRegistry::Helper {
  TypeRegistry* self{};

  template <typename T, T... Values>
  void RegisterSequence(std::integer_sequence<T, Values...>) {
    using dummy_list = int[];
    (void) dummy_list{(
        self->Register(
            std::type_index(
                typeid(std::integral_constant<T, Values>)).hash_code(),
            py::make_tuple(
                py::eval(std::to_string(Values))),
            std::to_string(Values)
          )
        , 0)...};
  }
};


template <typename TForm, typename T, T... Values>
auto transform(TForm = {}, std::integer_sequence<T, Values...> = {}) {
  return std::integer_sequence<T, TForm::template type<Values>::value...>{};
}

template <int Add>
struct int_add {
  template <int Value>
  using type = std::integral_constant<int, Value + Add>;
};

void TypeRegistry::RegisterCommon() {
  // Make mappings for C++ RTTI to Python types.
  // Unfortunately, this is hard to obtain from `pybind11`.
  RegisterType<bool>(eval("bool,"));
  RegisterType<std::string>(eval("str,"));
  RegisterType<double>(eval("float, np.double, ctypes.c_double"));
  RegisterType<float>(eval("np.float32, ctypes.c_float"));
  RegisterType<int>(eval("int, np.int32, ctypes.c_int32"));
  RegisterType<uint32_t>(eval("np.uint32, ctypes.c_uint32"));
  RegisterType<int64_t>(eval("np.int64, ctypes.c_int64"));

  Helper h{this};
  h.RegisterSequence(std::integer_sequence<bool, 0, 1>{});
  h.RegisterSequence(
      transform(
          int_add<-100>{}, std::make_integer_sequence<int, 200>{}));
}

py::object TypeRegistry::eval(const std::string& expr) const {
  return py::eval(expr, globals_, locals_);
}

void TypeRegistry::exec(const std::string& expr) {
  py::exec(expr, globals_, locals_);
}
