#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <random>
#include <chrono>

const int NUM_FILOSOFOS = 5;
std::vector<std::mutex> garfos(NUM_FILOSOFOS);
std::vector<char> estados(NUM_FILOSOFOS, 'P');

std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
std::uniform_int_distribution<int> dist_tempo(1, 1000);

void pensar(int filosofo) {
    std::this_thread::sleep_for(std::chrono::milliseconds(dist_tempo(rng)));
    estados[filosofo] = 'P';
}

void pegar_garfos(int filosofo) {
    int garfo_esquerda = filosofo;
    int garfo_direita = (filosofo + 1) % NUM_FILOSOFOS;
    
    garfos[garfo_esquerda].lock();
    garfos[garfo_direita].lock();
    
    estados[filosofo] = 'C';
}

void largar_garfos(int filosofo) {
    int garfo_esquerda = filosofo;
    int garfo_direita = (filosofo + 1) % NUM_FILOSOFOS;
    
    garfos[garfo_esquerda].unlock();
    garfos[garfo_direita].unlock();
    
    estados[filosofo] = 'P';
}

void filosofo(int filosofo) {
    while (true) {
        pensar(filosofo);
        std::cout << estados[0] << estados[1] << estados[2] << estados[3] << estados[4] << std::endl;
        pegar_garfos(filosofo);
        std::cout << estados[0] << estados[1] << estados[2] << estados[3] << estados[4] << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(dist_tempo(rng)));
        largar_garfos(filosofo);
    }
}

int main() {
    std::vector<std::thread> filosofos(NUM_FILOSOFOS);
    
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        filosofos[i] = std::thread(filosofo, i);
    }
    
    for (int i = 0; i < NUM_FILOSOFOS; i++) {
        filosofos[i].join();
    }
    
    return 0;
}
