#pragma once

#include "DicomLoadService.h"
#include "LoadWorker.h"
#include <QThread>
#include <QLabel>


DicomLoadService::DicomLoadService(QObject* parent) : QObject(parent) {}
DicomLoadService::~DicomLoadService() {
    cleanupThread(); // 객체가 소멸할 때 실행 중인 스레드가 있다면 완전히 종료될 때까지 대기
}
void DicomLoadService::cleanupThread() {
    if (m_thread) {
        if (m_thread->isRunning()) {
            m_thread->requestInterruption(); // 작업 중단 요청 (필요 시)
            m_thread->quit();
            m_thread->wait(); // ★ 스레드가 완전히 끝날 때까지 여기서 안전하게 대기!
        }
        delete m_thread; // 스레드가 완전히 멈춘 후 안전하게 메모리 해제
        m_thread = nullptr;
        m_worker = nullptr; // worker는 thread의 finished 시점에 deleteLater로 처리
    }
}
void DicomLoadService::loadAsync(const QString& folderPath) {

    cleanupThread();

    m_thread = new QThread(this);
    m_worker = new LoaderWorker(folderPath);

    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::started, m_worker, &LoaderWorker::run);
    connect(m_worker, &LoaderWorker::finished, this, [=](vtkSmartPointer<vtkImageData> data) {
        emit finished(data);
        m_thread->quit();
        });
    connect(m_worker, &LoaderWorker::errorOccurred, this, [=](QString msg) {
        emit error(msg);
        m_thread->quit();
        });


    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    

    m_thread->start();
}


//connect(thread, &QThread::started, worker, &LoaderWorker::run);
//
//스레드가 실제로 구동(start())되면, 자동으로 워커의 run() 함수가 실행되도록 연결합니다.
//
//## connect(worker, &LoaderWorker::finished, ...); &errorOccurred
//
//워커가 작업을 성공(finished)하거나 실패(errorOccurred)했을 때, 결과를 외부로 전달(emit)하고 스레드를 멈추는(thread->quit()) 동작을 연결합니다.
//
//connect(thread, &QThread::finished, ... deleteLater); (정리 예약)
//
//worker->deleteLater: 스레드가 완전히 끝났을 때 워커 메모리를 자동 해제합니다.
//
//thread->deleteLater : 스레드 객체 자신도 자동 해제합니다.
//
//this->deleteLater : (현재 코드를 품고 있는 서비스 객체 등) 작업이 끝난 주체도 안전하게 해제되도록 예약합니다.