# HexChat Python Module Style Guide

(This is a work in progress).

## General rules

- PEP8 as general fallback recommendations
- Max line length: 120
- Avoid overcomplex compound statements. i.e. dont do this: `somevar = x if x == y else z if a == b and c == b else x`

## Indentation style

### Multi-line functions

```python
foo(really_long_arg_1,
    really_long_arg_2)
```

### Mutli-line lists/dicts

```python
foo = {
    'bar': 'baz',
}
```
