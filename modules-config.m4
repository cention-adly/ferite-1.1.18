echo ""
echo "Configuring ferite modules..."
echo ""
modules=""
echo "checking module array ..."
sinclude(modules/array/config.m4)
echo "checking module console ..."
sinclude(modules/console/config.m4)
echo "checking module date ..."
sinclude(modules/date/config.m4)
echo "checking module filesystem ..."
sinclude(modules/filesystem/config.m4)
echo "checking module ipc ..."
sinclude(modules/ipc/config.m4)
echo "checking module math ..."
sinclude(modules/math/config.m4)
echo "checking module network ..."
sinclude(modules/network/config.m4)
echo "checking module number ..."
sinclude(modules/number/config.m4)
echo "checking module posix ..."
sinclude(modules/posix/config.m4)
echo "checking module reflection ..."
sinclude(modules/reflection/config.m4)
echo "checking module regexp ..."
sinclude(modules/regexp/config.m4)
echo "checking module serialize ..."
sinclude(modules/serialize/config.m4)
echo "checking module stream ..."
sinclude(modules/stream/config.m4)
echo "checking module string ..."
sinclude(modules/string/config.m4)
echo "checking module sys ..."
sinclude(modules/sys/config.m4)
echo "checking module test ..."
sinclude(modules/test/config.m4)
echo "checking module thread ..."
sinclude(modules/thread/config.m4)
echo "checking module unix ..."
sinclude(modules/unix/config.m4)
echo "checking module xml ..."
sinclude(modules/xml/config.m4)
AC_SUBST(modules)
echo ""
