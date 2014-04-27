import os

dir = raw_input("Root directory: ")
if not dir.endswith( "/" ):
    dir += "/"

prefix = raw_input("Enter prefix under %s: " % dir)

path = dir + prefix

if os.path.exists( path ):
    with open("gen_vpc.txt", "w") as out:
        for root, dirs, files in os.walk( path ):
            root = root.replace( dir, "" )
            files.sort()
            for file in files:
                out.write( "$File \"%s/%s\"\n" % (root, file) )
    print "VPC output sent to 'gen_vpc.txt'"
else:
        print "Folder %s not found!" % path
