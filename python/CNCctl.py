import serial


def displayhelp():
    print("List of commands:")
    print("help -- display help")
    print("exit -- quit controller")
    print("shell -- interactive G-code shell")


def shell():
    print("Entering Interactive G-code Shell...")
    print("Type return to return to controller")
    while True:
        code = input("G>")
        if(code == "return"):
            print("returning to normal shell");
            return

print("Welcome to CNC430 Contorl Program!")
print("type in commands to operate; help to list commands")

while True:
    comm = input(">")
    if(comm == "exit"):
        print()
        print("bye bye")
        exit()
    elif(comm == "help"):
        displayhelp()
    elif(comm == "shell"):
        shell()
