pipeline {
    agent {
        label 'vagrant'
    }
    stages {
        stage('Build') {
            steps {
                echo 'Start building polycube'
                sh 'cd ${POLYCUBE_WORKSPACE} && export JENKINS_RUNNING=true && ./start.sh ${WORKSPACE}'
            }
        }
    }

    post {
        always {
            sh 'rm -rf ${WORKSPACE}/*'
        }
    }
}
