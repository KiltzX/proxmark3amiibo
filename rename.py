import os

for fileName in os.listdir('amiibo/Zelda'):
   os.rename(r'amiibo/Zelda/' + fileName, r'amiibo/Zelda/' + fileName[0:-4] + '.eml')