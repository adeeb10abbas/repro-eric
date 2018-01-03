#!/usr/bin/env python

import inspect
import types

# Template definitions.
from pymodule.tpl.cpp_types import type_names, types_canonical

_PARAM_DEFAULT = 'first_registered'


def _get_module_name_from_stack(frame=2):
    return inspect.getmodule(inspect.stack()[frame][0]).__name__


def init_or_get(scope, name, template_cls, *args, **kwargs):
    tpl = getattr(scope, name, None)
    if tpl is None:
        if isinstance(scope, type):
            module_name = scope.__module__
        else:
            module_name = scope.__name__
        tpl = template_cls(name, *args, module_name=module_name, **kwargs)
        setattr(scope, name, tpl)
    return tpl


class Template(object):
    def __init__(self, name, param_default=_PARAM_DEFAULT, module_name=None):
        self.name = name
        if param_default and param_default != _PARAM_DEFAULT:
            param_default = types_canonical(param_default)
        self._param_default = param_default
        self.param_list = []
        # @note Consider using `StrictMap` if literals must exactly match.
        # (e.g. `0` and `False` should resolve to different instantiations).
        self._instantiation_map = {}
        if module_name is None:
            module_name = _get_module_name_from_stack()
        self._module_name = module_name

    def _param_resolve(self, param):
        # Resolve from default argument to canonical parameters.
        if param is None:
            assert self._param_default is not None
            param = self._param_default
        elif not isinstance(param, tuple):
            param = tuple(param)
        return types_canonical(param)

    def __getitem__(self, param):
        """Gets concrete class associate with the given arguments. """
        if not isinstance(param, tuple) or isinstance(param, list):
            # Handle scalar case.
            param = (param,)
        return self.get_instantiation(param)

    def get_instantiation(self, param=None, throw_error=True):
        """Gets the instantiation for the given parameters. """
        param = self._param_resolve(param)
        if throw_error:
            return self._instantiation_map[param]
        else:
            return self._instantiation_map.get(param)

    def _get_instantiation_name(self, param):
        return '{}[{}]'.format(self.name, ', '.join(type_names(param)))

    def add_instantiation(self, param, instantiation):
        """ Adds instantiation. """
        # Ensure that we do not already have this tuple.
        param = types_canonical(param)
        assert param not in self._instantiation_map, "Instantiation already registered"
        # Add it.
        self.param_list.append(param)
        self._instantiation_map[param] = instantiation
        if self._param_default == _PARAM_DEFAULT:
            self._param_default = param
        return param

    def add_instantiations(
            self, instantiation_func, param_list=None):
        assert param_list is not None
        for param in param_list:
            self.add_instantiation(param, instantiation_func(param))

    def _full_name(self):
        return "{}.{}".format(self._module_name, self.name)

    def __str__(self):
        cls_name = type(self).__name__
        return "<{} {}>".format(cls_name, self._full_name())


def is_instantiation_of(obj, tpl):
    # TODO: Return parameters for a given instantiation?
    return obj in tpl._instantiation_map.values()


class TemplateClass(Template):
    def add_instantiation(self, param, cls):
        """ Adds instantiation. """
        param = Template.add_instantiation(self, param, cls)
        # Update class information.
        cls._is_tpl = True
        cls.__name__ = self._get_instantiation_name(param)
        return param


def is_class_instantiation(obj):
    # Dunno how to register `_is_tpl` in methods...
    if isinstance(obj, type):
        return hasattr(obj, '_is_tpl')
    else:
        return False


class TemplateFunction(Template):
    pass


class TemplateMethod(TemplateFunction):
    def __init__(self, name, cls, **kwargs):
        TemplateFunction.__init__(self, name, **kwargs)
        self._cls = cls

    def __str__(self):
        return '<unbound TemplateMethod {}>'.format(self._full_name())

    def _full_name(self):
        return '{}.{}'.format(self._cls.__name__, self.name)

    def __get__(self, obj, objtype):
        # Descriptor accessor.
        if obj is None:
            return self
        else:
            return TemplateMethod._Bound(self, obj)

    def __set__(self, obj, value):
        raise RuntimeError("Read-only property")

    class _Bound(object):
        def __init__(self, tpl, obj):
            self._tpl = tpl
            self._obj = obj

        def __getitem__(self, param):
            unbound = self._tpl[param]
            bound = types.MethodType(unbound, self._obj, self._tpl._cls)
            return bound

        def __str__(self):
            return '<bound TemplateMethod {} of {}>'.format(
                self._tpl._full_name(), self._obj)
