import matplotlib.pyplot as plt

# Copier http://192.168.0.20/stats ici :
arrayRaw = [19.88,4,2,0,19.88,4,6,0]

start = 4*0
print(len(arrayRaw))
arrayRaw = arrayRaw[start:len(arrayRaw)]
print(len(arrayRaw))

temperatures = arrayRaw[0:len(arrayRaw):4]
heures = arrayRaw[1:len(arrayRaw):4]
chauffe = arrayRaw[3:len(arrayRaw):4]

plt.subplot(2,1,1)
plt.plot(temperatures, label='temperature (°C)')
plt.legend()

plt.subplot(2,1,2)
plt.plot(heures, label='heure')
plt.plot(chauffe, label='chauffe')
plt.legend()


plt.show()

