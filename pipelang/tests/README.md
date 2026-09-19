# PipeLang tests

Basic acceptance tests are wired through CTest in the top-level `CMakeLists.txt`.

```bash
cd build
ctest --output-on-failure
```

Add new `.pipe` files under `examples/` and register them as `add_test(...)`
entries. Later phases will add:

- invalid programs (expected failure)
- boundary cases (empty pipeline, single statement)
- semantic errors (undeclared dataset, type mismatch in filter predicate)
