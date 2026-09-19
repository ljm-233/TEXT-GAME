for f in $(find . -path ./.git -prune -o -type f \( \
  -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.c' -o \
  -name '*.cc' -o -name '*.glsl' -o -name '*.frag' -o -name '*.vert' -o \
  -name '*.cmake' -o -name 'CMakeLists.txt' -o -name '*.md' -o \
  -name '*.json' -o -name '*.yml' -o -name '*.yaml' -o -name '*.toml' \
\) -print); do
  echo "===== $f ====="
  cat "$f"
done > project_dump.txt
