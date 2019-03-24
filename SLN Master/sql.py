import csv, pyodbc

# set up some constants
MDB = 'C:/Users/Benedikt/Documents/GitLab/Schwering-Lighting-Network/SLN Master/master.mdb'; DRV = '{Microsoft Access Driver (*.mdb)}'; PWD = ''


def InputToUniversum(UID, Port):
    # connect to db
    con = pyodbc.connect('DRIVER={};DBQ={};PWD={}'.format(DRV,MDB,PWD))
    cur = con.cursor()

    # run a query and get the results 
    SQL = 'SELECT SLNUniversum FROM InputToUniversum WHERE UID='+str(UID)+' AND Port='+str(Port)+';' # your query goes here
    rows = cur.execute(SQL).fetchall()
    cur.close()
    con.close()

    return rows

def UniversumToOutput(SLNUniversum):
    # connect to db
    con = pyodbc.connect('DRIVER={};DBQ={};PWD={}'.format(DRV,MDB,PWD))
    cur = con.cursor()

    # run a query and get the results 
    SQL = 'SELECT UID, Port FROM UniversumToOutput WHERE SLNUniversum='+str(SLNUniversum)+';' # your query goes here
    rows = cur.execute(SQL).fetchall()
    cur.close()
    con.close()

    return rows

def Inputs():
    # connect to db
    con = pyodbc.connect('DRIVER={};DBQ={};PWD={}'.format(DRV,MDB,PWD))
    cur = con.cursor()

    # run a query and get the results 
    SQL = 'SELECT * FROM Inputs;' # your query goes here
    rows = cur.execute(SQL).fetchall()
    cur.close()
    con.close()

    return rows

def Outputs():
    # connect to db
    con = pyodbc.connect('DRIVER={};DBQ={};PWD={}'.format(DRV,MDB,PWD))
    cur = con.cursor()

    # run a query and get the results 
    SQL = 'SELECT * FROM Outputs;' # your query goes here
    rows = cur.execute(SQL).fetchall()
    cur.close()
    con.close()

    return rows

def getIP(UID):
    # connect to db
    con = pyodbc.connect('DRIVER={};DBQ={};PWD={}'.format(DRV,MDB,PWD))
    cur = con.cursor()

    # run a query and get the results 
    SQL = 'SELECT IP FROM Inputs WHERE UID='+str(UID)+';' # your query goes here
    rows = cur.execute(SQL).fetchall()
    SQL = 'SELECT IP FROM Outputs WHERE UID='+str(UID)+';' # your query goes here
    rows.append(cur.execute(SQL).fetchall())
    cur.close()
    con.close()

    return rows    