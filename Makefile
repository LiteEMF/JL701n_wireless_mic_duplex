# 总的 Makefile，用于调用目录下各个子工程对应的 Makefile
# 注意： Linux 下编译方式：
# 1. 从 http://pkgman.jieliapp.com/doc/all 处找到下载链接
# 2. 下载后，解压到 /opt/jieli 目录下，保证
#   /opt/jieli/common/bin/clang 存在（注意目录层次）
# 3. 确认 ulimit -n 的结果足够大（建议大于8096），否则链接可能会因为打开文件太多而失败
#   可以通过 ulimit -n 8096 来设置一个较大的值
# 支持的目标
# make ac701n
# make adapter_duplex

.PHONY: all clean ac701n ac701n_LiteEMF adapter_duplex clean_ac701n ac701n_LiteEMF clean_adapter_duplex

all: ac701n ac701n_LiteEMF adapter_duplex
	@echo +ALL DONE

clean: clean_ac701n  clean_ac701n_LiteEMF clean_adapter_duplex
	@echo +CLEAN DONE

ac701n:
	$(MAKE) -C apps/adapter/board/br28 -f Makefile

clean_ac701n:
	$(MAKE) -C apps/adapter/board/br28 -f Makefile clean

ac701n_LiteEMF:
	$(MAKE) -C apps/adapter/board/br28 -f Makefile_LiteEMF

clean_ac701n_LiteEMF:
	$(MAKE) -C apps/adapter/board/br28 -f Makefile_LiteEMF clean

adapter_duplex:
	$(MAKE) -C apps/adapter/board/br30 -f Makefile

clean_adapter_duplex:
	$(MAKE) -C apps/adapter/board/br30 -f Makefile clean
