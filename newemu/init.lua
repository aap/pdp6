function o(n)
	return tonumber(tostring(n), 8)
end
function fw(w)
	local l = (w >> 18) & o(777777)
	local r = w & o(777777)
	return oct(l,6) .. ",," .. oct(r,6)
end

--uxmount(1, "t/system.dtr");
uxmount(1, "t/systemdis.dtr");
uxmount(2, "t/its138.dtr");
uxmount(3, "t/music.dtr");


-- MACDMP RIM loader
d(o(00), o(255000000000), 0)
d(o(01), o(205000255000), 0)
d(o(02), o(700200635550), 0)
d(o(03), o(700600011577), 0)
d(o(04), o(721200223110), 0)
d(o(05), o(720200004010), 0)
d(o(06), o(720340001000), 0)
d(o(07), o(254000000006), 0)
d(o(10), o(720040000013), 0)
d(o(11), o(345540000006), 0)
d(o(12), o(602540777777), 0)
d(o(13), o(000000000013), 0)
d(o(14), o(254000000006), 0)

-- PTR RIM loader
d(o(20), o(710600000060))
d(o(21), o(710740000010))
d(o(22), o(254000000021))
d(o(23), o(710440000026))
d(o(24), o(710740000010))
d(o(25), o(254000000024))
d(o(27), o(254000000021))

--setpc(readmemory("t/ddt.16k.oct"))

ptrmount("t/hello.rim")
ptpmount("out.ptp")
ttycon()
