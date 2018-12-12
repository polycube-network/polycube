pipeline {
    agent {
        label 'vagrant'
    }
    stages {
        stage('Build') {
            steps {
                echo 'Start building polycube'
                sh 'cd ${POLYCUBE_WORKSPACE} && ./start.sh ${WORKSPACE}'
            }
        }
    }

    post {
        always {
            sh 'rm -rf ${WORKSPACE}/*'
        }
    }
}
