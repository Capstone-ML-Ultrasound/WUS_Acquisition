addpath('C:/Users/usbio-admin12/Desktop/emgintegrated/')
cd('C:/Users/usbio-admin12/Desktop/journal/')
addpath('C:\Users\usbio-admin12\Desktop\GestureRecognition\multi_finger_gestures\pinches\index\')
%% params
clear all
fs = 30e6;
fr = 1.2e3; % frame rate
dtype = 'uint16';
%% load
n_samples = 2272; % depth samples
root_sample = '2025.03.22_17.05.48';
filenameA = [root_sample '_1.A.bin']; 
rfsigA = readDSO(filenameA, dtype, n_samples, []);
%%
figure
imagesc(imgaussfilt(log10(abs(hilbert(rfsigA(:,:)-mean(rfsigA(:,:),1)))),[2 5]))
title('free')
