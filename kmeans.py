import numpy as np
import cv2
import random
import sys
import moviepy.editor as mpy

K_GROUPS = 8
NUM_ITERATIONS = 300

img = cv2.imread(r"C:\Users\Quentin\Pictures\Earth\4.jpg")
img = cv2.imread(r"C:\Users\Quentin\Desktop\test4.png")
img = np.ones((img.shape[0], img.shape[1], K_GROUPS, 3)) * np.reshape(img, (img.shape[0], img.shape[1], 1, 3))

frames = []

# Initialize groups
k_means = np.ones((1,1,K_GROUPS,img.shape[3]))
for i in range(K_GROUPS):
    k_means[0][0][i] = np.array([random.uniform(0, 255),random.uniform(0, 255),random.uniform(0, 255)])
k_means = np.ones(img.shape) * k_means

# Build up animation map
animations = {}
for i in range(K_GROUPS):
    animations[i] = None

for i in range(NUM_ITERATIONS):
    # Run k-means iteration
    choices = np.argmin(np.sqrt(np.sum((k_means-img)**2, axis=3)), axis=2)
    
    # Get averages
    avg = np.zeros((K_GROUPS,3))
    for j in range(K_GROUPS):
        total = img[np.where(choices == j)]
        if (len(total) != 0):
            avg[j] = np.sum(total, axis=0)[j] / len(total)
        else:
            avg[j] = np.array([0,0,0])

    # Update k-means
    new_k = np.ones((1,1,K_GROUPS,img.shape[3]))
    for j in range(K_GROUPS):
        if animations[j] != None:
            new_k[0][0][j] = k_means[0][0][j] + (0.08 * (animations[j][0] - k_means[0][0][j]))
            animations[j] = (animations[j][0], animations[j][1]-1)
            if (animations[j][1] == 0):
                animations[j] = None
        elif sum(avg[j]) != 0:
            new_k[0][0][j] = k_means[0][0][j] + (0.04 * (avg[j] - k_means[0][0][j]))
        else:
            new_k[0][0][j] = k_means[0][0][j]
    k_means = np.ones(img.shape) * new_k
    
    # Create new image
    new_img = np.zeros((img.shape[0], img.shape[1], 3))
    for j in range(K_GROUPS):
        indices = np.where(choices == j)
        new_img[indices] = np.rint(k_means[0][0][j]).astype(int)    
    
    new_img = cv2.cvtColor(np.uint8(new_img), cv2.COLOR_BGR2RGB)
    #new_img = cv2.GaussianBlur(new_img,(3,3),0)
    frames.append(new_img)
    
    # Check for new animations
    ANIMATION_TIME = 30
    if (i % ANIMATION_TIME == 0):
        ks = np.random.choice(K_GROUPS, size=3, replace=False)
        for k in ks:
            rand_color = np.array([random.uniform(0, 255),random.uniform(0, 255),random.uniform(0, 255)])
            animations[k] = (rand_color, ANIMATION_TIME)
        
    print("Frame " + str(i) + " done")
    sys.stdout.flush()

# Convert to numpy array
clip = mpy.ImageSequenceClip(frames, fps=30)
clip.write_gif("test4.gif", fps=30)

'''
cv2.imshow("asdasd", new_img)   
cv2.waitKey(0)
cv2.destroyAllWindows()
'''