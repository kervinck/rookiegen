import sys

posCount, posTotal = 0, 0
pos, count = None, 0

for line in sys.stdin:
        newPos, newCount = line.split(',')
        if newPos != pos:
                if count > 0:
                        posCount += 1
                        posTotal += count
                        print '%s,%d' % (pos, count)
                pos, count = newPos, 0
        count += int(newCount)

if count > 0:
        posCount += 1
        posTotal += count
        print '%s,%d' % (pos, count)

print >>sys.stderr, posCount, posTotal
