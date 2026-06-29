// SPDX-License-Identifier: MPL-2.0
// EN: Golden-image test — render the showcase, capture a PPM, compare against a stored reference.
//     First run: generates the reference and exits 0 (with a diagnostic).
//     Subsequent runs: compare via MSE and fail if the score exceeds the tolerance.
// PT: Teste de golden-image — renderiza o showcase, captura PPM, compara com referência armazenada.
//     1ª execução: gera a referência e sai 0 (com diagnóstico).
//     Execuções seguintes: compara via MSE e falha se a pontuação superar a tolerância.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include "capture.hpp"
#include <cstdio>
#include <vector>
#include <cstdlib>
static bool read_ppm(const char* p, std::vector<unsigned char>& d,int& w,int& h){
  FILE* f=fopen(p,"rb"); if(!f) return false; char m[3]={0};
  if(fscanf(f,"%2s %d %d %*d%*c",m,&w,&h)!=3){fclose(f);return false;}
  d.resize((size_t)w*h*3); fread(d.data(),1,d.size(),f); fclose(f); return true;
}
int main(){
  glintfx::App app({ .title="golden", .width=900, .height=600 });
  app.load("showcase.rml");

  // EN: Warm-up frames: let RmlUi layout stabilize before capture.
  // PT: Frames de aquecimento: deixa o layout do RmlUi estabilizar antes da captura.
  app.poll_events(); app.update(); app.render();
  app.poll_events(); app.update(); app.render();

  // EN: Capture frame using snapshot() which reads FBO 0 BEFORE glfwSwapBuffers.
  //     On Mesa/llvmpipe under Xvfb the back-buffer is undefined after swap, so we
  //     must capture here. snapshot() renders one frame, saves PPM, then swaps.
  // PT: Captura o frame usando snapshot() que lê o FBO 0 ANTES do glfwSwapBuffers.
  //     No Mesa/llvmpipe sob Xvfb o back-buffer é indefinido após o swap, então devemos
  //     capturar aqui. snapshot() renderiza um frame, salva PPM, depois troca.
  bool cap_ok = app.snapshot("out.ppm");
  fprintf(stderr,"[golden] snapshot -> %s\n", cap_ok ? "ok" : "FAIL");

  std::vector<unsigned char> ref,out; int rw,rh,ow,oh;
  if(!read_ppm("golden/showcase_reference.ppm",ref,rw,rh)){
    // 1ª execução: promove o output a referência e passa (com aviso)
    rename("out.ppm","golden/showcase_reference.ppm");
    std::puts("golden: referencia criada"); return 0;
  }
  read_ppm("out.ppm",out,ow,oh);
  if(rw!=ow||rh!=oh){ std::puts("golden: tamanho difere"); return 2; }
  long long sum=0; for(size_t i=0;i<out.size()&&i<ref.size();++i){ int d=out[i]-ref[i]; sum+=(long long)d*d; }
  double mse=(double)sum/out.size();
  printf("golden MSE=%.2f\n",mse);
  return mse < 50.0 ? 0 : 3;   // tolerância
}
