# EASTL部分

EASTL的子模块有环形依赖,绝对不要用submodule -r 递归。
唯一正确的克隆方式是
见 https://github.com/electronicarts/EASTL/commit/5bc06c380cac19395c42bb54e690ae2ee9f29ee1

```bash
git clone -b private git@github.com:BlurryLight/EASTL.git
cd EASTL
git submodule update --init
```
