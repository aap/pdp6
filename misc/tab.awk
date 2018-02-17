BEGIN {
	bayidx[0] = "A"
	bayidx[1] = "B"
	bayidx[2] = "C"
	bayidx[3] = "D"
	bayidx[4] = "E"
	bayidx[5] = "F"
	bayidx[6] = "H"
	bayidx[7] = "J"
	bayidx[8] = "K"
	bayidx[9] = "L"
	bayidx[10] = "M"
	bayidx[11] = "N"
}

{
	loc = $1
	cab = substr(loc, 1, 1)
	loc = substr(loc, 2)
	bay = index("ABCDEFHJKLMN", substr(loc, 1, 1))-1
	slot = substr(loc, 2)
	if(substr(slot, 1, 1) ~ /[ABCDEFHJKLMN]/){
		bay2 = index("ABCDEFHJKLMN", substr(slot, 1, 1))-1
		slot = substr(slot, 2)
		modules[cab][bay2][slot] = -1
	}
	modules[cab][bay][slot] = $2
}

END {
	cab = 1
	while(cab <= 4){
		print "<table border=\"1px\">"
		print "<tr>"
		print "<td></td>"
		j = 1
		while(j <= 25){
			print "<td>" j "</td>"
			j++
		}
		print "</tr>"
		for(i in bayidx){
			print "<tr>"
			print "<td>" bayidx[i] "</td>"
			j = 1
			while(j <= 25){
				x = modules[cab][i][j]
				if(modules[cab][i+1][j] == -1)
					print "<td rowspan=\"2\">" x "</td>"
				else if(x != -1)
					print "<td>" x "</td>"
				j++
			}
			print "</tr>"
		}
		print "</table>"
		cab++
	}
}
