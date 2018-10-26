class Shell:
    'CRTS shell command interface'
    count = 0

    def __init__(self, name, commandList=None):
        self.name = name
        Shell.count += 1

        if commandList:
            with open(commandList) as f:
                content = f.readlines()
                print content

    def help(self):
        print "Shell count %d" % Shell.count
