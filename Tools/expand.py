import chessmoves
import sys

for line in sys.stdin:
        pos, count = line.split(',')
        for newPos in chessmoves.moves(pos).values():
                print '%s,%s' % (newPos, count),
